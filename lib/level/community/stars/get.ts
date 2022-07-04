const getCommunityStars = () => {
	try {
		const value = localStorage.getItem('communityStars')
		if (!value) return {}

		const stars: unknown = JSON.parse(value)

		if (!(typeof stars === 'object' && stars))
			throw new Error('stars is not an object')

		if (Object.values(stars).some(count => typeof count !== 'number'))
			throw new Error('stars must be a mapping of strings to numbers')

		return stars as Record<string, number>
	} catch (error) {
		console.error(error)
		return {}
	}
}

export default getCommunityStars
