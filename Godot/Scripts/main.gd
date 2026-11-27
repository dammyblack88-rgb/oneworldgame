extends Node3D

const PLAYER_SCENE: PackedScene = preload("res://Scenes/Player.tscn")
var player: CharacterBody3D
var status_label: Label
var hint_label: Label
var hud_timer: float = 0.0

func _ready() -> void:
	_build_environment()
	_spawn_player()
	_build_hud()

func _process(delta: float) -> void:
	hud_timer += delta
	if hud_timer >= 0.25:
		hud_timer = 0.0
		_update_hud()

func _build_environment() -> void:
	var world_environment := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_SKY
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color(0.68, 0.72, 0.78)
	environment.ambient_light_energy = 0.65
	world_environment.environment = environment
	add_child(world_environment)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-48.0, -32.0, 0.0)
	sun.light_energy = 1.1
	add_child(sun)

	_make_box("Ground", Vector3(0, -0.25, 0), Vector3(90, 0.5, 90), Color(0.28, 0.34, 0.27), true)
	_make_box("Street", Vector3(0, 0.015, 0), Vector3(12, 0.04, 90), Color(0.12, 0.13, 0.14), false)
	_make_box("RoadMarking", Vector3(0, 0.04, 0), Vector3(0.18, 0.02, 90), Color(0.9, 0.82, 0.58), false)

	# Small, deliberately simple Lagos-inspired street blockout. Replace with authored assets later.
	var buildings := [
		[Vector3(-11, 2.5, -12), Vector3(9, 5, 10), Color(0.72, 0.48, 0.31)],
		[Vector3(-13, 3.0, 2), Vector3(7, 6, 9), Color(0.77, 0.68, 0.49)],
		[Vector3(12, 2.0, -13), Vector3(8, 4, 11), Color(0.62, 0.66, 0.57)],
		[Vector3(13, 3.5, 3), Vector3(9, 7, 10), Color(0.72, 0.57, 0.43)],
		[Vector3(-10, 2.0, 18), Vector3(10, 4, 8), Color(0.68, 0.59, 0.48)],
		[Vector3(11, 2.5, 19), Vector3(8, 5, 9), Color(0.75, 0.67, 0.54)]
	]
	for building in buildings:
		_make_box("BlockoutBuilding", building[0], building[1], building[2], true)

func _make_box(node_name: String, position: Vector3, size: Vector3, color: Color, solid: bool) -> void:
	var body: Node3D
	if solid:
		body = StaticBody3D.new()
	else:
		body = Node3D.new()
	body.name = node_name
	body.position = position
	add_child(body)

	var mesh_instance := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	mesh_instance.mesh = box
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	mesh_instance.material_override = material
	body.add_child(mesh_instance)

	if solid:
		var collision := CollisionShape3D.new()
		var shape := BoxShape3D.new()
		shape.size = size
		collision.shape = shape
		body.add_child(collision)

func _spawn_player() -> void:
	player = PLAYER_SCENE.instantiate() as CharacterBody3D
	player.position = Vector3(0, 0.1, 8)
	add_child(player)

func _build_hud() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)

	var panel := PanelContainer.new()
	panel.position = Vector2(18, 18)
	panel.custom_minimum_size = Vector2(310, 112)
	layer.add_child(panel)

	var column := VBoxContainer.new()
	panel.add_child(column)

	var title := Label.new()
	title.text = "ONE WORLD  |  LAGOS"
	title.add_theme_font_size_override("font_size", 20)
	column.add_child(title)

	status_label = Label.new()
	column.add_child(status_label)

	hint_label = Label.new()
	hint_label.text = "WASD move  •  SHIFT sprint  •  SPACE jump  •  ESC release mouse"
	hint_label.position = Vector2(18, 142)
	layer.add_child(hint_label)

func _update_hud() -> void:
	if not is_instance_valid(player):
		return
	var needs := player.get_node_or_null("Needs") as OneWorldNeeds
	var wallet := player.get_node_or_null("Economy") as OneWorldEconomy
	if needs and wallet:
		status_label.text = "Cash: ₦%s\nHunger %.0f  Energy %.0f  Hygiene %.0f\nSocial %.0f  Fun %.0f  Bladder %.0f" % [
			_format_money(wallet.balance),
			needs.get_need("Hunger"), needs.get_need("Energy"), needs.get_need("Hygiene"),
			needs.get_need("Social"), needs.get_need("Fun"), needs.get_need("Bladder")
		]

func _format_money(amount: int) -> String:
	var raw := str(amount)
	var output := ""
	for index in range(raw.length()):
		output += raw[index]
		var remaining := raw.length() - index - 1
		if remaining > 0 and remaining % 3 == 0:
			output += ","
	return output
