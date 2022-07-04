<script lang="ts">
	import MAX_STARS from '$lib/scene/star/max'

	import starImage from '../../../images/star.png'

	export let href: string
	export let disabled = false
	export let stars: number | null
</script>

<a {href} aria-disabled={disabled || undefined}>
	<slot />
	{#if stars !== null}
		<span>
			{#each { length: MAX_STARS } as _star, index (index)}
				<img
					src={starImage}
					alt="Star"
					data-hit={index < stars ? '' : undefined}
				/>
			{/each}
		</span>
	{/if}
</a>

<style lang="scss">
	a {
		display: flex;
		position: relative;
		justify-content: center;
		align-items: center;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s, opacity 0.3s;

		&:hover {
			color: white;
		}
	}

	[aria-disabled] {
		pointer-events: none;
		opacity: 0.5;
	}

	span {
		position: absolute;
		top: 0.5rem;
		right: 0.7rem;
	}

	img {
		$size: 1.3rem;

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
