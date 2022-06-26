import levels from '../levels'

const getStars = () => {
	try {
		const value = localStorage.getItem('stars')
		if (!value) return []

		const stars: unknown = JSON.parse(value)

		if (!Array.isArray(stars)) throw new Error('stars is not an array')

		if (stars.length > levels.length)
			throw new Error('stars has more elements than levels')

		if (stars.some(count => typeof count !== 'number'))
			throw new Error('stars must be an array of numbers')

		return stars as number[]
	} catch (error) {
		console.error(error)
		return []
	}
}

export default getStars
