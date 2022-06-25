import type Position from '$lib/position'
import distanceSquared from './squared'

const distance = (a: Position, b: Position) => Math.sqrt(distanceSquared(a, b))

export default distance
