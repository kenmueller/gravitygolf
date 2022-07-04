<script lang="ts">
	import mobile from '$lib/mobile'
	import totalStars from '$lib/level/stars/total'
	import levels from '$lib/level/levels'
	import MetaImage from '../../../components/Meta/Image.svelte'
	import MetaTitle from '../../../components/Meta/Title.svelte'
	import MetaDescription from '../../../components/Meta/Description.svelte'
	import BackLink from '../../../components/Link/Back.svelte'
	import Level from '../../../components/Levels/Level.svelte'
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
		{#each levels as level, index (level)}
			<Level id={index + 1} />
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
