import type RawLevel from '$lib/level/raw'

export default interface EditorSceneOptions {
	publishLink: string | null
	initialData: RawLevel | null
}
