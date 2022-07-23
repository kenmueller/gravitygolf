export default interface Snapshot {
	id: string
	exists: boolean | (() => boolean)
	get(key: string): unknown
}
