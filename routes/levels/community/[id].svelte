<script context="module" lang="ts">
	export const load: Load = ({ fetch, params }) => _load(fetch, params.id)
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'

	import type CommunityLevel from '$lib/level/community'
	import CommunityLevelPage, {
		load as _load
	} from '../../../components/Level/Page/Community.svelte'

	export let level: CommunityLevel
</script>

<CommunityLevelPage {level} />
