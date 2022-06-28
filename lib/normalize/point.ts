import type Position from '$lib/position'

const normalizePoint = <Point extends Position>(
	point: Point,
	bounds: { width: number; height: number },
	center: Position
): Point => ({
	...point,
	x: point.x + bounds.width / 2 + center.x,
	y: -point.y + bounds.height / 2 - center.y
})

export default normalizePoint
