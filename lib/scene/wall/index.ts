import type Position from '$lib/position'

export default interface Wall extends Position {
	width: number
	height: number
}
