const replaceWithRounded = (places: number) => {
	const scale = Math.pow(10, places)

	return (_key: string, value: unknown) =>
		typeof value === 'number' ? Math.round(value * scale) / scale : value
}

export default replaceWithRounded
