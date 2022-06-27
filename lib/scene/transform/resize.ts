import type View from '$lib/view'

const resize = (canvas: HTMLCanvasElement, view: View) => {
	canvas.width = Math.floor(view.width * view.scale)
	canvas.height = Math.floor(view.height * view.scale)
	canvas.style.width = `${view.width}px`
	canvas.style.height = `${view.height}px`
}

export default resize
