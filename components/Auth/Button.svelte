<script lang="ts">
	import currentUser from '$lib/user/current'
	import _signIn from '$lib/user/sign/in'
	import errorFromValue from '$lib/error/from/value'
	import User from '../../images/User.svelte'

	let loading = false

	const signIn = async () => {
		try {
			if ($currentUser || loading) return
			loading = true

			await _signIn()
		} catch (value) {
			console.error(value)
			alert(errorFromValue(value).message)
		} finally {
			loading = false
		}
	}
</script>

<button
	aria-busy={loading || undefined}
	disabled={Boolean($currentUser)}
	on:click={$currentUser ? undefined : signIn}
>
	<User />
	<span>{$currentUser ? $currentUser.name || 'Anonymous' : 'Sign in'}</span>
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
		transition: color 0.3s, opacity 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			height: 2rem;
		}
	}

	[aria-busy] {
		pointer-events: none;
		opacity: 0.5;
	}

	span {
		margin: 0 1.2rem 0 1rem;
	}
</style>
