import type Force from './force'

export default interface SceneEvents {
	forces: [number, number]
	stars: [number]
	force: [Force | null]
}
