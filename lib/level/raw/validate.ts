import type RawLevel from '.'
import MAX_STARS from '$lib/scene/star/max'

const KEYS = ['gravity', 'antigravity', 'ball', 'hole', 'stars', 'walls']

const isArray = (value: unknown, type: string, length: number) =>
	Array.isArray(value) &&
	value.length === length &&
	value.every(element => typeof element === type)

const checkForces = (level: RawLevel, type: 'gravity' | 'antigravity') => {
	if (!(type in level)) return true
	const value = level[type]

	return (
		(typeof value === 'number' && value >= 0) ||
		(Array.isArray(value) &&
			typeof value[0] === 'number' &&
			value.slice(1).every(force => isArray(force, 'number', 2)))
	)
}

const checkObjects = (
	level: RawLevel,
	type: 'stars' | 'walls',
	objectLength: number,
	maxLength: number | null = null
) => {
	if (!(type in level)) return true
	const value = level[type]

	return (
		Array.isArray(value) &&
		(maxLength === null || value.length <= maxLength) &&
		(value as unknown[]).every(object =>
			isArray(object, 'number', objectLength)
		)
	)
}

const validateRawLevel = (data: unknown): data is RawLevel => {
	const level = data as RawLevel

	return (
		typeof level === 'object' &&
		level !== null &&
		Object.keys(level).every(key => KEYS.includes(key)) &&
		checkForces(level, 'gravity') &&
		checkForces(level, 'antigravity') &&
		isArray(level.ball, 'number', 3) &&
		isArray(level.hole, 'number', 3) &&
		checkObjects(level, 'stars', 3, MAX_STARS) &&
		checkObjects(level, 'walls', 4)
	)
}

export default validateRawLevel
