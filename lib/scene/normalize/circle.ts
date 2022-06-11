const normalizeCircle = <Circle extends { x: number; y: number }>(
	circle: Circle,
	bounds: { width: number; height: number }
): Circle => ({
	...circle,
	x: circle.x + bounds.width / 2,
	y: -circle.y + bounds.height / 2
})

export default normalizeCircle
