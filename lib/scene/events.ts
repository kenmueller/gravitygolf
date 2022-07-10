import type Force from './force'

export default interface SceneEvents {
	hit: [boolean]
	forces: [number, number]
	stars: [number]
	force: [Force | null]
	attempt: []
	win: []
}
