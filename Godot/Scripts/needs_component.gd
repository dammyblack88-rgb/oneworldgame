extends Node
class_name OneWorldNeeds

signal need_changed(need_name: String, value: float)

const NEED_NAMES := ["Hunger", "Energy", "Hygiene", "Social", "Fun", "Bladder"]
@export var decay_per_minute: Dictionary = {
	"Hunger": 1.2,
	"Energy": 0.8,
	"Hygiene": 0.5,
	"Social": 0.35,
	"Fun": 0.4,
	"Bladder": 1.0
}
var values: Dictionary = {}

func _ready() -> void:
	for need_name in NEED_NAMES:
		values[need_name] = 100.0

func _process(delta: float) -> void:
	for need_name in NEED_NAMES:
		var decay: float = float(decay_per_minute.get(need_name, 0.0))
		set_need(need_name, float(values.get(need_name, 100.0)) - decay * delta / 60.0)

func set_need(need_name: String, value: float) -> void:
	if not values.has(need_name):
		return
	var updated := clampf(value, 0.0, 100.0)
	if not is_equal_approx(float(values[need_name]), updated):
		values[need_name] = updated
		need_changed.emit(need_name, updated)

func restore_need(need_name: String, amount: float) -> void:
	if values.has(need_name):
		set_need(need_name, float(values[need_name]) + amount)

func get_need(need_name: String) -> float:
	return float(values.get(need_name, 0.0))

func get_lowest_need() -> String:
	var lowest_name := ""
	var lowest_value := 101.0
	for need_name in NEED_NAMES:
		var current := get_need(need_name)
		if current < lowest_value:
			lowest_value = current
			lowest_name = need_name
	return lowest_name
