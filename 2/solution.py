from __future__ import annotations
from dataclasses import dataclass
from enum import Enum, unique
from functools import total_ordering
from types import MappingProxyType
from typing import Mapping, Tuple, Union

@total_ordering
@unique
class OpponentMove(Enum):
  """Opponent move class."""
  ROCK = "A"
  PAPER = "B"
  SCISSOR = "C"

  def __hash__(self):
    return hash(self.value)

  def __eq__(self, other: SelfMove):
    if self is OpponentMove.ROCK:
      return other is SelfMove.ROCK
    elif self is OpponentMove.PAPER:
      return other is SelfMove.PAPER
    elif self is OpponentMove.SCISSOR:
      return other is SelfMove.SCISSOR
    else:
      raise ValueError(f"Invalid opponent move: {self}")
  
  def __lt__(self, other: SelfMove):
    if self is OpponentMove.ROCK:
      return other is SelfMove.PAPER
    elif self is OpponentMove.PAPER:
      return other is SelfMove.SCISSOR
    elif self is OpponentMove.SCISSOR:
      return other is SelfMove.ROCK
    else:
      raise ValueError(f"Invalid opponent move: {self}")

@unique
class SelfMove(Enum):
  """Self move class."""
  ROCK = "X"
  PAPER = "Y"
  SCISSOR = "Z"

  def __hash__(self):
    return hash(self.value)

  def __eq__(self, other: OpponentMove):
    return other == self
  
  def __lt__(self, other: OpponentMove):
    return not other <= self

MoveType = Union[OpponentMove, SelfMove]

@unique
class Outcome(Enum):
  """Game outcome."""
  WIN = "Z"
  DRAW = "Y"
  LOSS = "X"

  @classmethod
  def create(cls, move1: MoveType, move2: MoveType) -> Outcome:
    if type(move1) is type(move2):
      raise RuntimeError("Moves should be of different types.")
    self_move = move1 if type(move1) is SelfMove else move2
    opponent_move = move1 if self_move == move2 else move2

_MOVE_SCORES: Mapping[SelfMove, int] = MappingProxyType({SelfMove.ROCK: 1, SelfMove.PAPER: 2, SelfMove.SCISSOR: 3})
_OUTCOME_SCORES: Mapping[Outcome, int] = MappingProxyType({Outcome.LOSS: 0, Outcome.DRAW: 3, Outcome.WIN: 6})
_OPPONENT_OUTCOME_MAP: Mapping[Tuple[OpponentMove, Outcome], SelfMove] = MappingProxyType(
  {
    (OpponentMove.PAPER, Outcome.DRAW): SelfMove.PAPER,
    (OpponentMove.PAPER, Outcome.LOSS): SelfMove.ROCK,
    (OpponentMove.PAPER, Outcome.WIN): SelfMove.SCISSOR,

    (OpponentMove.ROCK, Outcome.DRAW): SelfMove.ROCK,
    (OpponentMove.ROCK, Outcome.LOSS): SelfMove.SCISSOR,
    (OpponentMove.ROCK, Outcome.WIN): SelfMove.PAPER,

    (OpponentMove.SCISSOR, Outcome.DRAW): SelfMove.SCISSOR,
    (OpponentMove.SCISSOR, Outcome.LOSS): SelfMove.PAPER,
    (OpponentMove.SCISSOR, Outcome.WIN): SelfMove.ROCK,
  }
)

@dataclass(frozen=True)
class Strategy1:
  opponent_move: OpponentMove
  self_move: SelfMove

  @classmethod
  def create_from_line(cls, strategy: str) -> Strategy1:
    strategy = strategy.strip()
    moves = strategy.split()
    opponent_move = moves[0]
    self_move = moves[1]
    return Strategy1(OpponentMove(opponent_move), SelfMove(self_move))
  
  @property
  def score(self) -> Outcome:
    outcome: Outcome 
    if self.self_move < self.opponent_move:
      outcome = Outcome.LOSS
    elif self.self_move == self.opponent_move:
      outcome = Outcome.DRAW
    else:
      outcome = Outcome.WIN
    return _OUTCOME_SCORES[outcome] + _MOVE_SCORES[self.self_move]

@dataclass(frozen=True)
class Strategy2:
  opponent_move: OpponentMove
  desired_outcome: Outcome

  @classmethod
  def create_from_line(cls, strategy: str) -> Strategy2:
    strategy = strategy.strip()
    strategy_parts = strategy.split()
    opponent_move = strategy_parts[0]
    desired_outcome = strategy_parts[1]
    return Strategy2(OpponentMove(opponent_move), Outcome(desired_outcome))

  @property
  def score(self) -> Outcome:
    return Strategy1(self.opponent_move, _OPPONENT_OUTCOME_MAP[(self.opponent_move, self.desired_outcome)]).score

with open("input", "r") as input_file:
  strategy_lines = input_file.readlines()
  strategies_1 = [Strategy1.create_from_line(strategy_line) for strategy_line in strategy_lines]
  score_1 = sum(strategy.score for strategy in strategies_1)
  print(f"Problem 1: {score_1}")

  strategies_2 = [Strategy2.create_from_line(strategy_line) for strategy_line in strategy_lines]
  score_2 = sum(strategy.score for strategy in strategies_2)
  print(f"Problem 2: {score_2}")
