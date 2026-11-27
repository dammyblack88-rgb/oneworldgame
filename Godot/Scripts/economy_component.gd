extends Node
class_name OneWorldEconomy

signal balance_changed(new_balance: int)

@export var starting_balance: int = 5000
var balance: int = 0

func _ready() -> void:
	balance = maxi(0, starting_balance)
	balance_changed.emit(balance)

func can_afford(amount: int) -> bool:
	return amount >= 0 and balance >= amount

func try_spend(amount: int) -> bool:
	if amount < 0 or not can_afford(amount):
		return false
	balance -= amount
	balance_changed.emit(balance)
	return true

func add_funds(amount: int) -> void:
	if amount <= 0:
		return
	balance += amount
	balance_changed.emit(balance)
