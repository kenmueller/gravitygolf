import type Force from './force'
import type Star from './star'
import type Wall from './wall'

export default interface SceneEvents {
	hit: [boolean]
	forces: [number, number]
	stars: [number]
	force: [Force | null]
	star: [Star | null]
	wall: [Wall | null]
	attempt: []
	win: []
}
