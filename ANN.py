import torch
import torch.nn as nn


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


if __name__ == '__main__':
    model = ChessANN()
    print(model)

    total_params = sum(p.numel() for p in model.parameters())
    print(f"\nTotal parameters: {total_params:,}")

    # test forward pass
    x = torch.randn(4, 789)   # batch of 4 positions
    out = model(x)
    print(f"Input shape:  {x.shape}")
    print(f"Output shape: {out.shape}")
    print(f"Output range: [{out.min().item():.4f}, {out.max().item():.4f}]")
