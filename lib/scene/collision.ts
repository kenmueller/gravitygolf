import distance from "./distance";

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
	// const halfWidth = rectangle.width / 2
	// const halfHeight = rectangle.height / 2
	const circleRight = circle.x + circle.radius
	const circleLeft = circle.x - circle.radius
	const circleBottom = circle.y + circle.radius
	const circleTop = circle.y - circle.radius
	const rectRight = rectangle.x + rectangle.width
	const rectBottom = rectangle.y + rectangle.height

	const leftDiff = circleRight - rectangle.x
	const rightDiff = rectRight - circleLeft
	const topDiff = circleTop - rectangle.y
	const bottomDiff = rectBottom - circleBottom
	// if (
	// 	circle.x + circle.radius < rectangle.x ||
	// 	circle.x - circle.radius > rectangle.x + rectangle.width ||
	// 	circle.y + circle.radius < rectangle.y ||
	// 	circle.y - circle.radius > rectangle.y + rectangle.height
	// )
	// 	return null

	if (leftDiff < 0 || rightDiff < 0 || topDiff < 0 || bottomDiff < 0)
		return null

	if (
		(leftDiff >= 0 &&
			rightDiff >= 0 &&
			circle.y > rectangle.y &&
			circle.y < rectBottom) ||
		(topDiff >= 0 &&
			bottomDiff >= 0 &&
			circle.x > rectangle.x &&
			circle.x < rectRight)
	)
		return likelyDirection(leftDiff, rightDiff, topDiff, bottomDiff)
	
	if (distance(circle, rectangle) <= circle.radius)

	// const distX = Math.abs(circle.x - rectangle.x - rectangle.width / 2)
	// const distY = Math.abs(circle.y - rectangle.y - rectangle.height / 2)

	// if (distX > rectangle.width / 2 + circle.radius) return null
	// if (distY > rectangle.height / 2 + circle.radius) return null

	// if (distX <= rectangle.width / 2) return 'horizontal'
	// if (distY <= rectangle.height / 2) return 'vertical'

	// const deltaX = distX - rectangle.width / 2
	// const deltaY = distY - rectangle.height / 2

	// return deltaX ** 2 + deltaY ** 2 <= circle.radius ** 2
}

export default collision
