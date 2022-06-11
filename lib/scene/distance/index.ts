import distanceSquared from './squared'

const distance = (a: { x: number; y: number }, b: { x: number; y: number }) =>
	Math.sqrt(distanceSquared(a, b))

export default distance
