const normalizeRectangle = <
	Rectangle extends { x: number; y: number; width: number; height: number }
>(
	rectangle: Rectangle,
	bounds: { width: number; height: number }
): Rectangle => ({
	...rectangle,
	x: rectangle.x + (bounds.width - rectangle.width) / 2,
	y: -rectangle.y + (bounds.height - rectangle.height) / 2
})

export default normalizeRectangle
