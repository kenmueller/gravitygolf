<script context="module" lang="ts">
	export const load: Load = ({ params }) => _load(params.id)
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import MainLevelPage, {
		load as _load
	} from '../../components/Level/Page/Main.svelte'

	export let id: number
</script>

<MainLevelPage {id} />
