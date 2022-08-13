<script lang="ts" context="module">
	export const load: Load = async ({ fetch }) => {
		try {
			if (browser) {
				isInitial.set(true)
				initialLevels.set(await getLevels(fetch, get(query)))
			}

			return {}
		} catch (value) {
			const { code, message } = errorFromValue(value)
			return { status: code, error: message }
		}
	}
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'
	import { get } from 'svelte/store'

	import isInitial from '$lib/level/community/initial'
	import query from '$lib/level/community/query'
	import initialLevels from '$lib/level/community/levels/initial'
	import getLevels from '$lib/level/community/levels/get'
	import errorFromValue from '$lib/error/from/value'
	import mobile from '$lib/mobile'
	import totalStars from '$lib/level/community/stars/total'
	import levels from '$lib/level/community/levels'
	import MetaImage from '../../../components/Meta/Image.svelte'
	import MetaTitle from '../../../components/Meta/Title.svelte'
	import MetaDescription from '../../../components/Meta/Description.svelte'
	import BackLink from '../../../components/Link/Back.svelte'
	import Search from '../../../components/Search.svelte'
	import Level from '../../../components/Level/Cell/Community.svelte'
	import Edit from '../../../images/Edit.svelte'
	import { browser } from '$app/env'
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
		<a href="/levels/editor">
			<Edit />
			Create Level
		</a>
		<span class="stars" data-stars={$totalStars} />
	</header>
	<div>
		<Search placeholder="Community Levels" focus bind:value={$query} />
	</div>
	<div class="levels">
		{#each $levels ?? $initialLevels as level (level.id)}
			<Level {level} />
		{/each}
	</div>
</main>

<style lang="scss">
	main {
		@include scroll.bar;

		display: flex;
		flex-direction: column;
		align-items: center;
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
		align-items: center;
	}

	a {
		display: flex;
		align-items: center;
		margin-left: auto;
		white-space: nowrap;
		color: white;
		opacity: 0.7;
		transition: opacity 0.3s;

		&:hover {
			opacity: 1;
		}

		> :global(svg) {
			width: 1.8rem;
			margin-right: 0.5rem;
		}
	}

	.stars {
		$radius: 1rem;

		position: relative;
		height: 2 * $radius;
		width: 2 * $radius;
		margin: 0 1.3rem 0 2rem;
		background-image: url('../../../images/star.png');
		background-size: contain;
		background-repeat: no-repeat;

		&::after {
			content: attr(data-stars);
			position: absolute;
			top: 0;
			right: 0;
			padding: 0 0.4rem;
			color: rgba(black, 0.5);
			background: rgba(white, 0.6);
			border-radius: 0.5rem;
			transform: translate(1.35rem, -1.15rem);
		}
	}

	div {
		margin-top: 1.5rem;
	}

	.levels {
		display: grid;
		grid-auto-rows: 8rem;
		grid-template-columns: repeat(auto-fit, minmax(15rem, 1fr));
		gap: 1.5rem;

		@media (min-width: 58rem) {
			gap: 2rem;
		}
	}
</style>
