const distanceSquared = (
	a: { x: number; y: number },
	b: { x: number; y: number }
) => (b.x - a.x) ** 2 + (b.y - a.y) ** 2

export default distanceSquared
