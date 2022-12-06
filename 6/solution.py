from typing import Final, Optional

_SOM_UNIQUE_LEN: Final = 14
_SOP_UNIQUE_LEN: Final = 4

def detect_marker(datastream: str, unique_len: int) -> int:
  entry_sequence = [None] * unique_len
  entry_map = {}
  for idx, data in enumerate(datastream):
    to_remove: Optional[int] = None
    if idx >= unique_len:
      to_remove = entry_sequence[idx % unique_len]

    entry_sequence[idx % unique_len] = data
    if data not in entry_map:
      entry_map[data] = 1
    else:
      entry_map[data] += 1
    
    if to_remove is not None:
      entry_map[to_remove] -= 1
      if entry_map[to_remove] == 0:
        entry_map.pop(to_remove)
    
    if len(entry_map) == unique_len:
      return idx + 1
  return 0


datastream: str
with open("input", "r") as input_file:
  strategy_lines = input_file.readlines()
  strategy_lines = [line.strip() for line in strategy_lines]
  datastream = strategy_lines[0]

print(f"Problem 1: {detect_marker(datastream, _SOP_UNIQUE_LEN)}")
print(f"Problem 2: {detect_marker(datastream, _SOM_UNIQUE_LEN)}")
