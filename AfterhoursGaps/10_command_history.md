# Command History (Undo/Redo)

## Working Implementation
See these files for a complete working example:
- `src/extracted/command_history.h` - Clean, standalone implementation ready for PR
- `src/editor/text_buffer.h` - Domain-specific usage in TextBuffer

## Problem
Afterhours does not provide a generic undo/redo system based on the Command pattern.

## Use Cases
- **Level editors**: Undo place/delete/move entity
- **Paint/drawing tools**: Undo brush strokes
- **Form inputs**: Undo text changes
- **Game tools**: Undo any reversible action
- **Replay systems**: Commands can be serialized for replay

## Current Workaround
Custom `CommandHistory` class in `src/editor/text_buffer.h` with:
- `Command` base class with `execute()` and `undo()` methods
- Undo/redo stacks
- Specific commands: `InsertCharCommand`, `DeleteCharCommand`, `DeleteSelectionCommand`

## Proposed API

```cpp
namespace afterhours {

/// Base class for undoable commands
class Command {
public:
  virtual void execute() = 0;
  virtual void undo() = 0;
  virtual std::string description() const { return "Command"; }
  
  // Optional: merge consecutive similar commands (e.g., typing)
  virtual bool can_merge_with(const Command& other) const { return false; }
  virtual void merge_with(Command& other) {}
};

/// Command history manager
class CommandHistory {
public:
  void execute(std::unique_ptr<Command> cmd);  // Execute and push
  void push(std::unique_ptr<Command> cmd);     // Push without executing
  void undo();
  void redo();
  
  bool can_undo() const;
  bool can_redo() const;
  
  std::string next_undo_description() const;  // For UI: "Undo: Move entity"
  std::string next_redo_description() const;
  
  void clear();
  void set_max_history(std::size_t max);
};

/// Lambda-based command for simple cases
std::unique_ptr<Command> make_command(
    std::function<void()> do_fn,
    std::function<void()> undo_fn,
    std::string desc = "Action");

} // namespace afterhours
```

## Usage Example

```cpp
CommandHistory history;

// Simple lambda command
int value = 10;
history.execute(make_command(
    [&]() { value = 20; },
    [&]() { value = 10; },
    "Set value"
));

history.undo();  // value = 10
history.redo();  // value = 20

// Complex command with class
class MoveEntityCommand : public Command {
  Entity& entity_;
  Vector2 from_, to_;
public:
  MoveEntityCommand(Entity& e, Vector2 from, Vector2 to)
    : entity_(e), from_(from), to_(to) {}
  void execute() override { entity_.position = to_; }
  void undo() override { entity_.position = from_; }
  std::string description() const override { return "Move entity"; }
};
```

## Notes
- Useful for any application with reversible actions
- Command merging allows grouping (e.g., all keystrokes become one "Insert text")
- History size limiting prevents memory bloat
- Descriptions enable helpful UI: "Edit > Undo Move entity"

