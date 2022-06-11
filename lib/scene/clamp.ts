const clamp = (min: number, max: number, value: number) =>
	Math.min(max, Math.max(min, value))

export default clamp
