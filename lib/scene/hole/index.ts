import type Position from '$lib/position'
import type ImageRef from '$lib/image/ref'

export default interface Hole extends Position {
	radius: number
	image: ImageRef
}
