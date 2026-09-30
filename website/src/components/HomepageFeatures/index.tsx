import type { ReactNode } from 'react';
import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

type FeatureItem = {
  title: string;
  description: ReactNode;
};

const FeatureList: FeatureItem[] = [
  {
    title: 'A sky from a seed',
    description: (
      <>
        A seed makes the whole composition: the nebula and its lighting, the
        galaxy and where in it you are, its stars and clusters. The same seed is
        always the same sky, and a small project file makes it again anywhere.
      </>
    ),
  },
  {
    title: 'Steered by words, not parameters',
    description: (
      <>
        Thirteen controls describe the sky -- open or enveloping, luminous or
        brooding, galactic or remote -- each confirmed blind to do what its name
        says. The model&rsquo;s eighty raw parameters stay out of the way.
      </>
    ),
  },
  {
    title: 'Judged where it is seen',
    description: (
      <>
        Every change to the look is decided by a blind comparison scored by eye,
        at a game&rsquo;s field of view -- never by a number, and never by a
        whole-sky map no player will see.
      </>
    ),
  },
  {
    title: 'HDR for any engine',
    description: (
      <>
        OpenEXR, KTX2 and PNG, as faces, a cross or an equirectangular map,
        turned to face your scene, with the direction of the sky&rsquo;s key
        light written beside it for your sun to match.
      </>
    ),
  },
];

function Feature({ title, description }: FeatureItem) {
  return (
    <div className={clsx('col col--6')}>
      <div className={styles.feature}>
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures(): ReactNode {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
