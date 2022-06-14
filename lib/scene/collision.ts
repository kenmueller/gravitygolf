import distance from './distance'

const likelyDirection = (
	left: number,
	right: number,
	top: number,
	bottom: number
) =>
	left < right && left < top && left < bottom
		? 0
		: right < top && right < bottom
		? Math.PI
		: top < bottom
		? (3 * Math.PI) / 2
		: Math.PI / 2

const collision = (
	circle: { x: number; y: number; radius: number },
	rectangle: { x: number; y: number; width: number; height: number }
) => {
	const circleRight = circle.x + circle.radius
	const circleLeft = circle.x - circle.radius
	const circleBottom = circle.y + circle.radius
	const circleTop = circle.y - circle.radius
	const rectRight = rectangle.x + rectangle.width
	const rectBottom = rectangle.y + rectangle.height

	const leftDiff = circleRight - rectangle.x
	const rightDiff = rectRight - circleLeft
	const topDiff = circleBottom - rectangle.y
	const bottomDiff = rectBottom - circleTop

	// Most of the time there's no collision so first see if there's possibly a collision

	if (leftDiff < 0 || rightDiff < 0 || topDiff < 0 || bottomDiff < 0)
		return null
	// Next see if there's a collision on a side

	// console.log(
	// 	'collision',
	// 	circle,
	// 	rectangle,
	// 	leftDiff,
	// 	rightDiff,
	// 	topDiff,
	// 	bottomDiff,
	// 	rectBottom
	// )
	if (
		(leftDiff >= 0 &&
			rightDiff >= 0 &&
			circle.y >= rectangle.y &&
			circle.y <= rectBottom) ||
		(topDiff >= 0 &&
			bottomDiff >= 0 &&
			circle.x >= rectangle.x &&
			circle.x <= rectRight)
	)
		return likelyDirection(leftDiff, rightDiff, topDiff, bottomDiff)

	// Finally check if the collision is on a corner

	if (distance(circle, rectangle) <= circle.radius)
		return Math.atan2(circle.y - rectangle.y, rectangle.x - circle.x)

	if (distance(circle, { x: rectRight, y: rectangle.y }) <= circle.radius)
		return Math.atan2(circle.y - rectangle.y, rectRight - circle.x)

	if (distance(circle, { x: rectangle.x, y: rectBottom }) <= circle.radius)
		return Math.atan2(circle.y - rectBottom, rectangle.x - circle.x)

	if (distance(circle, { x: rectRight, y: rectBottom }) <= circle.radius)
		return Math.atan2(circle.y - rectBottom, rectRight - circle.x)

	return null
}

export default collision
