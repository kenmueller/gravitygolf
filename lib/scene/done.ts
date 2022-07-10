export default interface LevelDone {
	setStars(stars: number): void
	hasNext: boolean
	next: string
}
