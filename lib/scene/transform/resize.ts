const resize = (canvas: HTMLCanvasElement) => {
	const scale = window.devicePixelRatio

	canvas.width = Math.floor(window.innerWidth * scale)
	canvas.height = Math.floor(window.innerHeight * scale)
	canvas.style.width = `${window.innerWidth}px`
	canvas.style.height = `${window.innerHeight}px`
}

export default resize
