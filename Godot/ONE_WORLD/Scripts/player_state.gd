extends Node
class_name OneWorldPlayerState

signal needs_changed(needs: Dictionary)
signal balance_changed(new_balance: int)

const NEED_NAMES := [&"hunger", &"energy", &"hygiene", &"social", &"fun", &"bladder"]

@export var starting_balance: int = 5000
@export var decay_per_minute: Dictionary = {
	&"hunger": 1.2,
	&"energy": 0.8,
	&"hygiene": 0.5,
	&"social": 0.35,
	&"fun": 0.45,
	&"bladder": 1.0
}

var needs: Dictionary = {
	&"hunger": 100.0,
	&"energy": 100.0,
	&"hygiene": 100.0,
	&"social": 100.0,
	&"fun": 100.0,
	&"bladder": 100.0
}
var balance: int = 0

func _ready() -> void:
	balance = maxi(0, starting_balance)
	balance_changed.emit(balance)

func _process(delta: float) -> void:
	var changed := false
	for need_name in NEED_NAMES:
		var rate: float = float(decay_per_minute.get(need_name, 0.0))
		var next_value: float = maxf(0.0, float(needs[need_name]) - rate * delta / 60.0)
		if not is_equal_approx(next_value, float(needs[need_name])):
			needs[need_name] = next_value
			changed = true
	if changed:
		needs_changed.emit(needs.duplicate(true))

func restore_need(need_name: StringName, amount: float) -> void:
	if not needs.has(need_name):
		return
	needs[need_name] = clampf(float(needs[need_name]) + amount, 0.0, 100.0)
	needs_changed.emit(needs.duplicate(true))

func set_need(need_name: StringName, value: float) -> void:
	if not needs.has(need_name):
		return
	needs[need_name] = clampf(value, 0.0, 100.0)
	needs_changed.emit(needs.duplicate(true))

func get_lowest_need() -> StringName:
	var lowest: StringName = NEED_NAMES[0]
	for need_name in NEED_NAMES:
		if float(needs[need_name]) < float(needs[lowest]):
			lowest = need_name
	return lowest

func can_afford(amount: int) -> bool:
	return amount >= 0 and balance >= amount

func add_funds(amount: int) -> void:
	if amount <= 0:
		return
	balance += amount
	balance_changed.emit(balance)

func try_spend(amount: int) -> bool:
	if amount < 0 or not can_afford(amount):
		return false
	balance -= amount
	balance_changed.emit(balance)
	return true
