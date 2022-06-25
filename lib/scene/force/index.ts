import type Position from '$lib/position'
import type ImageRef from '$lib/image/ref'

export default interface Force extends Position {
	direction: 1 | -1
	image: ImageRef
}
