<script lang="ts">
	import mobile from '$lib/mobile'
	import currentUser from '$lib/user/current'
	import _signOut from '$lib/user/sign/out'
	import deleteLevelProgress from '$lib/level/progress/remove'
	import errorFromValue from '$lib/error/from/value'
	import MetaImage from '../components/Meta/Image.svelte'
	import MetaTitle from '../components/Meta/Title.svelte'
	import MetaDescription from '../components/Meta/Description.svelte'
	import BackLink from '../components/Link/Back.svelte'
	import SignOut from '../images/SignOut.svelte'
	import Trash from '../images/Trash.svelte'

	let signOutLoading = false

	const signOut = async () => {
		try {
			if (!$currentUser || signOutLoading) return
			signOutLoading = true

			await _signOut()
		} catch (value) {
			console.error(value)
			alert(errorFromValue(value).message)
		} finally {
			signOutLoading = false
		}
	}

	const deleteProgress = () => {
		switch (prompt("Type 'delete' to delete all progress")) {
			case 'delete':
				deleteLevelProgress()
				alert('All progress was deleted')
				break
			case null:
				break
			default:
				alert("'delete' was typed incorrectly")
		}
	}
</script>

<MetaImage />
<MetaTitle value="Settings | Gravity Golf" />
<MetaDescription />

<main>
	<header>
		<BackLink href="/">
			Settings
			{#if !$mobile}| Gravity Golf{/if}
		</BackLink>
	</header>
	<div>
		{#if $currentUser}
			<button
				class="sign-out"
				aria-busy={signOutLoading || undefined}
				on:click={signOut}
			>
				<SignOut />
				<span>Sign out</span>
			</button>
		{/if}
		<button class="delete-progress" on:click={deleteProgress}>
			<Trash />
			<span>Delete all progress</span>
		</button>
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
		justify-content: space-between;
		align-items: center;
	}

	div {
		display: flex;
		flex-direction: column;
		align-items: center;
		margin-top: 1.5rem;
	}

	.sign-out,
	.delete-progress {
		display: flex;
		align-items: center;
		max-width: 20rem;
		width: 100%;
		margin-top: 2rem;
		padding: 0.5rem 0.5rem;
		font-size: 1rem;
		color: white;
		border-radius: 0.5rem;
		transition: opacity 0.3s;

		&:not([aria-busy]):hover {
			opacity: 0.7;
		}

		> :global(svg) {
			flex-shrink: 0;
			width: 2rem;
		}

		> span {
			margin: 0 auto;
		}
	}

	.sign-out {
		background: #ee9f0b;

		> :global(svg) {
			transform: translateY(0.05rem);
		}
	}

	.delete-progress {
		background: #de4b46;
	}

	[aria-busy] {
		pointer-events: none;
		opacity: 0.5;
	}
</style>
