"""
NNUE Training Pipeline for bbc.c

Usage:
    python train.py                         # train from data/positions.csv
    python train.py --data my_data.csv      # custom data file
    python train.py --epochs 50 --lr 1e-3   # custom hyperparams

After training:
    - Saves checkpoint to models/best.pt
    - Exports weights to models/weights.h  (C header, paste into bbc.c)
"""

import argparse
import csv
import math
import os
import struct
from pathlib import Path

import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader, random_split

# ── Feature encoding (789 dimensions) ────────────────────────────────────────
#
#   [0   .. 767]  piece-square occupancy: 12 pieces × 64 squares
#                 index = piece_type * 64 + square
#                 piece order: P N B R Q K p n b r q k  (white then black)
#   [768]         side to move: 1.0 = white, 0.0 = black
#   [769 .. 772]  castling rights: K Q k q
#   [773 .. 788]  en-passant square one-hot
#                 indices 773-780 → ep squares a3-h3 (black just pushed)
#                 indices 781-788 → ep squares a6-h6 (white just pushed)
#
# Total: 768 + 1 + 4 + 16 = 789

try:
    import chess
    HAS_CHESS = True
except ImportError:
    HAS_CHESS = False
    raise ImportError("python-chess is required: pip install chess")

PIECE_ORDER = [
    chess.PAWN, chess.KNIGHT, chess.BISHOP,
    chess.ROOK, chess.QUEEN, chess.KING,
]

# ep squares: rank 2 (index 2) = a3..h3, rank 5 (index 5) = a6..h6
EP_SQUARES = (
    [chess.square(f, 2) for f in range(8)] +   # a3-h3
    [chess.square(f, 5) for f in range(8)]      # a6-h6
)
EP_SQUARE_INDEX = {sq: i for i, sq in enumerate(EP_SQUARES)}

INPUT_SIZE  = 789
SCORE_SCALE = 600.0  # centipawns → tanh target:  target = tanh(score/scale)


def fen_to_features(fen: str) -> torch.Tensor:
    """Convert a FEN string to a 789-dimensional float32 feature vector."""
    board = chess.Board(fen)
    features = torch.zeros(INPUT_SIZE, dtype=torch.float32)

    # ── piece-square (0..767) ─────────────────────────────────────────────
    for color in (chess.WHITE, chess.BLACK):
        color_offset = 0 if color == chess.WHITE else 6
        for piece_idx, piece_type in enumerate(PIECE_ORDER):
            bb = board.pieces(piece_type, color)
            base = (color_offset + piece_idx) * 64
            for sq in bb:
                features[base + sq] = 1.0

    # ── side to move (768) ────────────────────────────────────────────────
    features[768] = 1.0 if board.turn == chess.WHITE else 0.0

    # ── castling rights (769..772) ────────────────────────────────────────
    features[769] = float(board.has_kingside_castling_rights(chess.WHITE))
    features[770] = float(board.has_queenside_castling_rights(chess.WHITE))
    features[771] = float(board.has_kingside_castling_rights(chess.BLACK))
    features[772] = float(board.has_queenside_castling_rights(chess.BLACK))

    # ── en passant (773..788) ─────────────────────────────────────────────
    ep_sq = board.ep_square
    if ep_sq is not None and ep_sq in EP_SQUARE_INDEX:
        features[773 + EP_SQUARE_INDEX[ep_sq]] = 1.0

    return features


def score_to_target(score_cp: float) -> float:
    """Normalise centipawn score to (-1, 1) using tanh."""
    return math.tanh(score_cp / SCORE_SCALE)


# ── Dataset ───────────────────────────────────────────────────────────────────

class PositionDataset(Dataset):
    def __init__(self, csv_path: str):
        self.samples: list[tuple[str, float]] = []
        with open(csv_path, newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                fen   = row["fen"]
                score = float(row["score"])
                self.samples.append((fen, score_to_target(score)))
        print(f"Loaded {len(self.samples):,} positions from {csv_path}")

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        fen, target = self.samples[idx]
        x = fen_to_features(fen)
        y = torch.tensor([target], dtype=torch.float32)
        return x, y


# ── Model ─────────────────────────────────────────────────────────────────────

class ChessANN(nn.Module):
    def __init__(self):
        super().__init__()
        self.network = nn.Sequential(
            nn.Linear(789, 512),
            nn.ReLU(),
            nn.Linear(512, 1),
            nn.Tanh(),  # output in [-1, 1]
        )

    def forward(self, x):
        return self.network(x)


# ── Training ──────────────────────────────────────────────────────────────────

def train(args):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Device: {device}")

    # dataset
    full_dataset = PositionDataset(args.data)
    val_size  = max(1, int(0.1 * len(full_dataset)))
    train_size = len(full_dataset) - val_size
    train_ds, val_ds = random_split(full_dataset, [train_size, val_size])

    train_loader = DataLoader(train_ds, batch_size=args.batch, shuffle=True,
                              num_workers=2, pin_memory=True)
    val_loader   = DataLoader(val_ds,   batch_size=args.batch, shuffle=False,
                              num_workers=2, pin_memory=True)

    # model
    model = ChessANN().to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, patience=3, factor=0.5, verbose=True
    )
    criterion = nn.MSELoss()

    Path("models").mkdir(exist_ok=True)
    best_val_loss = float("inf")

    for epoch in range(1, args.epochs + 1):
        # ── train ─────────────────────────────────────────────────────────
        model.train()
        train_loss = 0.0
        for x, y in train_loader:
            x, y = x.to(device), y.to(device)
            optimizer.zero_grad()
            pred = model(x)
            loss = criterion(pred, y)
            loss.backward()
            optimizer.step()
            train_loss += loss.item() * len(x)
        train_loss /= train_size

        # ── validate ──────────────────────────────────────────────────────
        model.eval()
        val_loss = 0.0
        with torch.no_grad():
            for x, y in val_loader:
                x, y = x.to(device), y.to(device)
                val_loss += criterion(model(x), y).item() * len(x)
        val_loss /= val_size

        scheduler.step(val_loss)

        print(f"Epoch {epoch:3d}/{args.epochs} | "
              f"train={train_loss:.6f}  val={val_loss:.6f}")

        if val_loss < best_val_loss:
            best_val_loss = val_loss
            torch.save(model.state_dict(), "models/best.pt")
            print("  → saved best model")

    print(f"\nTraining complete. Best val loss: {best_val_loss:.6f}")

    # load best weights before export
    model.load_state_dict(torch.load("models/best.pt", map_location="cpu"))
    export_weights(model)


# ── Weight export to C header ─────────────────────────────────────────────────

def export_weights(model: ChessANN):
    """
    Write models/weights.h — a C header with all layer weights/biases
    as float arrays.  Include this in bbc.c and call nnue_eval() instead
    of evaluate().
    """
    model.eval()
    layers = []
    for name, param in model.named_parameters():
        layers.append((name, param.detach().cpu().numpy()))

    lines = [
        "/* Auto-generated by train.py — do not edit */",
        "#pragma once",
        "",
    ]

    for name, arr in layers:
        flat = arr.flatten()
        cname = name.replace(".", "_")
        lines.append(f"static const float {cname}[{len(flat)}] = {{")
        # 8 values per line
        for i in range(0, len(flat), 8):
            chunk = flat[i:i+8]
            lines.append("    " + ", ".join(f"{v:.8f}f" for v in chunk) + ",")
        lines.append("};")
        lines.append(f"#define {cname.upper()}_SIZE {len(flat)}")
        lines.append("")

    # emit layer shapes as defines for easy C integration
    lines += [
        "/* Layer shapes */",
        "#define NNUE_INPUT  789",
        "#define NNUE_HIDDEN 512",
        "#define NNUE_OUTPUT 1",
        "",
        "/* Call nnue_eval() to replace evaluate() in bbc.c */",
        "static inline float nnue_eval(const float *input) {",
        "    static float h[512];",
        "    /* hidden layer: ReLU(W1 @ input + b1) */",
        "    for (int i = 0; i < 512; i++) {",
        "        float s = network_0_bias[i];",
        "        for (int j = 0; j < 789; j++)",
        "            s += network_0_weight[i * 789 + j] * input[j];",
        "        h[i] = s > 0.0f ? s : 0.0f;  /* ReLU */",
        "    }",
        "    /* output layer: tanh(W2 @ h + b2) */",
        "    float out = network_2_bias[0];",
        "    for (int i = 0; i < 512; i++)",
        "        out += network_2_weight[i] * h[i];",
        "    /* tanh approximation: output in (-1, 1) */",
        "    out = out < -10.0f ? -1.0f : out > 10.0f ? 1.0f",
        "        : (float)tanh(out);",
        "    /* convert back to centipawns: ~600 * atanh(out) */",
        "    return out * 600.0f;",
        "}",
    ]

    out_path = "models/weights.h"
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Weights exported to {out_path}")


# ── Entry point ───────────────────────────────────────────────────────────────

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--data",   default="data/positions.csv")
    parser.add_argument("--epochs", type=int,   default=30)
    parser.add_argument("--batch",  type=int,   default=2048)
    parser.add_argument("--lr",     type=float, default=1e-3)
    args = parser.parse_args()

    if not os.path.exists(args.data):
        raise FileNotFoundError(
            f"No data file at {args.data}\n"
            "Run generate_data.py first to create training data."
        )

    train(args)
