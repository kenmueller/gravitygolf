import type Position from '$lib/position'

const splitHypotenuse = (
	a: Position,
	b: Position,
	distance: number,
	hypotenuse: number
) => {
	const scale = hypotenuse / distance
	return { x: (a.x - b.x) * scale, y: (a.y - b.y) * scale }
}

export default splitHypotenuse
