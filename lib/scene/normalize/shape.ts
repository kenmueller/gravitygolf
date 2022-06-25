import type Position from '$lib/position'

const normalizeShape = <
	Shape extends Position &
		({ radius: number } | { width: number; height: number })
>(
	shape: Shape,
	bounds: { width: number; height: number },
	center: Position
): Shape => ({
	...shape,
	x:
		shape.x +
		bounds.width / 2 -
		('radius' in shape ? shape.radius : shape.width / 2) +
		center.x,
	y:
		-shape.y +
		bounds.height / 2 -
		('radius' in shape ? shape.radius : shape.height / 2) -
		center.y
})

export default normalizeShape
