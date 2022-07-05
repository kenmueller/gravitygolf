<script lang="ts">
	import type CommunityLevelRecord from '$lib/level/community/record'
	import stars from '$lib/level/community/stars'
	import Base from './Base.svelte'

	export let level: CommunityLevelRecord
</script>

<Base href="/levels/community/{level.id}" stars={$stars?.[level.id] ?? null}>
	{level.name}
</Base>
