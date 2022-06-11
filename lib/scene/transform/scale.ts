const scale = (context: CanvasRenderingContext2D) => {
	const scale = window.devicePixelRatio
	context.scale(scale, scale)
}

export default scale
