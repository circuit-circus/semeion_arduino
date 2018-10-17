class Animation {

  private:
    float t;
    float[][] curves;
    boolean cyclic;
    boolean curveEnded;
    boolean animationEnded;
    int i; 

  public:

    Animation::Animation(float[][] c) {
      curves = c;
    }

    float  Animation::animate() {

      float y = 0;

      t += (1 / duration);
      
      if (t > 1) {
        t = 0;
        animationEnded = true;

        if ( i < curves.size() - 1) {
          i++;
        } else {
          i = 0;
        }
      } else {
        animationEnded = false;
      }

      y = (1 - t) * (1 - t) * (1 - t) * p[i][0] + 3 * (1 - t) * (1 - t) * t * p[i][1] + 3 * (1 - t) * t * t * p[i][2] + t * t * t * p[i][3]; //Cubic Bezier

      return y;

    }

    boolean hasEnded() {
      return animationEnded;
    }

    boolean isCycling() {
      return cyclic;
    }

    Animation getNext() {
      return nextAnimation;
    }
};




