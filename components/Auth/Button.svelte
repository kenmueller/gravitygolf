<script lang="ts">
	import User from '../../images/User.svelte'
</script>

<button>
	<User />
	<span>Sign in</span>
</button>

<style lang="scss">
	$padding: 0.5rem;

	button {
		display: flex;
		align-items: center;
		padding: $padding 0 $padding $padding;
		font-size: 1rem;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			height: 2rem;
		}
	}

	span {
		margin: 0 1.2rem 0 1rem;
	}
</style>
