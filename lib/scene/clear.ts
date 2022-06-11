const clear = (
	canvas: HTMLCanvasElement,
	context: CanvasRenderingContext2D
) => {
	context.clearRect(0, 0, canvas.width, canvas.height)
}

export default clear
