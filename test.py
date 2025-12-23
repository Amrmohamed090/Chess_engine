import chess

# main.py
import ctypes
import os
import test_set

test_cases = test_set.parsed_test_cases
# Load the shared library
# Use os.path.abspath to ensure the correct path is used
lib_path = os.path.abspath("bbc.dll") 
bb = ctypes.CDLL(lib_path)

# Define the function prototype (optional but recommended for type safety)
bb.test_suit.argtypes = [ctypes.c_char_p] # Specifies argument types
bb.test_suit.restype = ctypes.c_int # Specifies return type

# # Call the C function from Python
# result = bb.add(4, 5)


board = chess.Board()

for id in test_cases.keys():
    fen = test_cases[id]
    board = chess.Board(fen)
    count = board.legal_moves.count()
    mycount = bb.test_suit(fen.encode("utf-8"))

    if count == mycount:
        print("passed")
    else:
        print(f"correct: {count}, predicted {mycount}, in fen : {fen}")
    



