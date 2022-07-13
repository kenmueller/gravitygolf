<script lang="ts">
	import MAX_STARS from '$lib/scene/star/max'

	import starImage from '../../images/star.png'

	export let stars: number
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
<slot />

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
</style>
