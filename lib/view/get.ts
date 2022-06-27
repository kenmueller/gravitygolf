import type View from '.'

const getView = (mobile: boolean): View => ({
	width: window.innerWidth,
	height: window.innerHeight,
	scale: window.devicePixelRatio,
	mobile
})

export default getView
