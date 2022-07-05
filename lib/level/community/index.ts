import type RawLevel from '../raw'

export default interface CommunityLevel {
	id: string
	name: string
	gravity: number
	antigravity: number
	stars: number
	attempts: number
	wins: number
	data: RawLevel
}
