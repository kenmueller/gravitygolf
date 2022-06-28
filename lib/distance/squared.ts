import type Position from '$lib/position'

const distanceSquared = (a: Position, b: Position) =>
	(b.x - a.x) ** 2 + (b.y - a.y) ** 2

export default distanceSquared
