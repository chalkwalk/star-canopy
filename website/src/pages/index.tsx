import type { ReactNode } from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import HomepageFeatures from '@site/src/components/HomepageFeatures';
import Heading from '@theme/Heading';

import styles from './index.module.css';

function HomepageHeader() {
  const { siteConfig } = useDocusaurusContext();
  return (
    <header className={clsx('hero', styles.heroBanner)}>
      <div className="container">
        {/* Decorative: the title below says the name already. */}
        <img src="img/logo.png" alt="" className={styles.heroLogo} />
        <Heading as="h1" className="hero__title">
          {siteConfig.title}
        </Heading>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <div className={styles.buttons}>
          <Link className="button button--primary button--lg" to="/docs/getting-started">
            Get started
          </Link>
          <Link className="button button--secondary button--lg" to="/docs/gallery">
            See the gallery
          </Link>
        </div>
        <div className={clsx('margin-top--lg', styles.heroScreenshot)}>
          <img
            src="img/hero.jpg"
            alt="A sky StarCanopy made, unrolled flat: a dark billowing nebula lit from within at the right, the galaxy's band of stars sweeping up from the left"
            className={styles.screenshotImage}
          />
        </div>
      </div>
    </header>
  );
}

export default function Home(): ReactNode {
  return (
    <Layout
      title="Procedural space skyboxes, baked on the GPU"
      description="StarCanopy generates skyboxes for space games: a lit nebula, the galaxy's band and its stars, as an HDR cubemap any engine can load -- from a seed, steered by controls that describe the sky.">
      <HomepageHeader />
      <main>
        <HomepageFeatures />
      </main>
    </Layout>
  );
}
