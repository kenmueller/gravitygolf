<script lang="ts" context="module">
	export const load: Load = async ({ fetch }) => {
		try {
			initialCommunityLevels.set(
				await getCommunityLevels(fetch, get(communityLevelsQuery))
			)

			return { props: {} }
		} catch (value) {
			const { code, message } = errorFromValue(value)
			return { status: code, error: message }
		}
	}
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'
	import { get } from 'svelte/store'

	import communityLevelsQuery from '$lib/level/community/query'
	import initialCommunityLevels from '$lib/level/community/levels/initial'
	import getCommunityLevels from '$lib/level/community/levels/get'
	import errorFromValue from '$lib/error/from/value'
	import mobile from '$lib/mobile'
	import totalStars from '$lib/level/stars/total'
	import levels from '$lib/level/community/levels'
	import MetaImage from '../../../components/Meta/Image.svelte'
	import MetaTitle from '../../../components/Meta/Title.svelte'
	import MetaDescription from '../../../components/Meta/Description.svelte'
	import BackLink from '../../../components/Link/Back.svelte'
	import Level from '../../../components/Level/Cell/Community.svelte'
</script>

<MetaImage />
<MetaTitle value="Community Levels | Gravity Golf" />
<MetaDescription />

<main>
	<header>
		<BackLink href="/">
			Community Levels
			{#if !$mobile}| Gravity Golf{/if}
		</BackLink>
		<span class="stars" data-stars={$totalStars} />
	</header>
	<div>
		{#each $levels as level (level.id)}
			<Level {level} />
		{/each}
	</div>
</main>

<style lang="scss">
	main {
		display: flex;
		flex-direction: column;
		align-items: center;
		overflow: hidden;
		height: 100%;
		overflow-y: auto;
		padding: 2rem 0;
	}

	header,
	div {
		max-width: 62.5rem;
		width: 95%;
	}

	header {
		display: flex;
		justify-content: space-between;
		align-items: center;
	}

	.stars {
		$radius: 1rem;

		position: relative;
		height: 2 * $radius;
		width: 2 * $radius;
		margin-right: 1.3rem;
		background-image: url('../../../images/star.png');
		background-size: contain;
		background-repeat: no-repeat;

		&::after {
			content: attr(data-stars);
			position: absolute;
			top: 0;
			right: 0;
			padding: 0 0.4rem;
			color: transparentize(black, 0.5);
			background: transparentize(white, 0.4);
			border-radius: 0.5rem;
			transform: translate(1.35rem, -1.15rem);
		}
	}

	div {
		display: grid;
		grid-auto-rows: 5rem;
		grid-template-columns: repeat(auto-fit, minmax(15rem, 1fr));
		gap: 1.5rem;
		margin-top: 1.5rem;

		@media (min-width: 58rem) {
			gap: 2rem;
		}
	}
</style>
