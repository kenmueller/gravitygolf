import type View from '$lib/view'

const scale = (context: CanvasRenderingContext2D, { scale }: View) => {
	context.scale(scale, scale)
}

export default scale
