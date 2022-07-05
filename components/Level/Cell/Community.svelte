<script lang="ts">
	import type CommunityLevelRecord from '$lib/level/community/record'
	import stars from '$lib/level/community/stars'
	import Base from './Base.svelte'

	export let level: CommunityLevelRecord
</script>

<Base href="/levels/community/{level.id}" stars={$stars?.[level.id] ?? null}>
	{level.name}
	<span>
		{level.attempts} attempt{level.attempts === 1 ? '' : 's'} •
		{level.wins} win{level.wins === 1 ? '' : 's'}
	</span>
</Base>

<style lang="scss">
	span {
		margin-top: 0.2rem;
		font-size: 0.9rem;
	}
</style>
