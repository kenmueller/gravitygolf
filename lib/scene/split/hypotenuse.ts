const splitHypotenuse = (
	a: { x: number; y: number },
	b: { x: number; y: number },
	distance: number,
	hypotenuse: number
) => {
	const scale = hypotenuse / distance
	return { x: (a.x - b.x) * scale, y: (a.y - b.y) * scale }
}

export default splitHypotenuse
