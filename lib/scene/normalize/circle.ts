import type Position from '$lib/position'

const normalizeCircle = <Circle extends Position>(
	circle: Circle,
	bounds: { width: number; height: number }
): Circle => ({
	...circle,
	x: circle.x + bounds.width / 2,
	y: -circle.y + bounds.height / 2
})

export default normalizeCircle
