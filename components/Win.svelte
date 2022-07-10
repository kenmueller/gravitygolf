<script lang="ts">
	import MAX_STARS from '$lib/scene/star/max'
	import hideOverlay from '$lib/overlay/hide'
	import RightArrow from '../images/RightArrow.svelte'

	import starImage from '../images/star.png'

	export let stars: number
	export let hasNext: boolean
	export let next: string
</script>

<h1>Congratulations!</h1>
<div class="stars">
	{#each { length: MAX_STARS } as _star, index (index)}
		<img
			class="star"
			src={starImage}
			alt="Star"
			data-hit={index < stars ? '' : undefined}
		/>
	{/each}
</div>
<a href={next} on:click={hideOverlay}>
	{#if hasNext}
		Next
		<RightArrow />
	{:else}
		Done
	{/if}
</a>

<style lang="scss">
	h1 {
		color: white;
	}

	.stars {
		margin-top: 2rem;
	}

	.star {
		$size: 3rem;

		pointer-events: none;
		width: $size;
		height: $size;
		opacity: 0.5;
		transition: opacity 0.3s;

		& + & {
			margin-left: 0.5rem;
		}
	}

	[data-hit] {
		opacity: 1;
	}

	a {
		display: flex;
		align-items: center;
		margin-top: 2.5rem;
		padding: 0.5rem 1rem;
		color: white;
		background: colors.$blue;
		border-radius: 0.5rem;
		transition: opacity 0.3s;

		> :global(svg) {
			height: 1.1rem;
			margin-left: 0.8rem;
		}

		&:hover {
			opacity: 0.7;
		}
	}
</style>
