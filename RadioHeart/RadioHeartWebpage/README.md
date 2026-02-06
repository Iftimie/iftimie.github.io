# Presence Landing Page

Single-scroll landing page for a couples product: a pair of softly glowing hearts.

## Files

- `index.html`
- `styles.css`
- `main.js`
- `netlify.toml`
- `assets/` media

## Asset slots (swap without editing layout)

- Hero video: `assets/video/hero.mp4`
- Hero poster: `assets/posters/hero-poster.jpg`
- Open Graph image: `assets/images/og-image.jpg`
- Product image: `assets/images/product-pair.jpg`
- Ritual image: `assets/images/ritual-hands.jpg`
- Scenes:
  - `assets/images/scene-bedside.jpg`
  - `assets/images/scene-parkbench.jpg`
  - `assets/images/scene-desk.jpg`
  - `assets/images/scene-home.jpg`

## Content map

- Hero: `index.html` (headline, subhead, scroll cue)
- Contrast: `index.html` (2 lines)
- Product truth: `index.html` (1 paragraph + image)
- Ritual: `index.html` (5 steps + image)
- Scenes: `index.html` (gallery + captions)
- What you get: `index.html` (4 short lines)
- Final CTA: `index.html` (Mailchimp form)

## Mailchimp free tier hookup

1. In Mailchimp, go to **Audience** -> **Signup forms** -> **Embedded forms**.
2. Copy the form action URL, it looks like:

   `https://XXXX.us17.list-manage.com/subscribe/post?u=AAA&id=BBB`

3. Open `main.js` and set:

- `MAILCHIMP_DC` to `us17`
- `MAILCHIMP_U` to `AAA`
- `MAILCHIMP_ID` to `BBB`

This site uses JSONP (`post-json`) so it works on static hosting without server-side code.

## Local preview

Open `index.html` directly, or run a tiny server:

```bash
python -m http.server 8080
```

## Netlify

- Publish directory: project root
- Build command: none

If you're deploying via GitHub:

1. Push this folder to a new GitHub repo.
2. In Netlify: **Add new site** -> **Import from Git**.
3. Pick the repo, then deploy (no build settings needed).
