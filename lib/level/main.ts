import type RawLevel from './raw'

export default interface MainLevel {
	id: string
	message: string | null
	data: RawLevel
}
