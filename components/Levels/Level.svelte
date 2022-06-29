<script lang="ts">
	import MAX_STARS from '$lib/scene/star/max'
	import stars from '$lib/level/stars'

	import starImage from '../../images/star.png'

	export let id: number

	$: starCount = $stars?.[id - 1] ?? null
</script>

<a
	href="/levels/{id}"
	aria-disabled={id - 1 > ($stars?.length ?? 0) || undefined}
>
	Level {id}
	{#if starCount !== null}
		<span>
			{#each { length: MAX_STARS } as _star, index}
				<img
					src={starImage}
					alt="Star"
					data-hit={index < starCount ? '' : undefined}
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
		text-decoration: none;
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
