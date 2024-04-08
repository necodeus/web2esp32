gsap
  .timeline({
    defaults: {
      duration: 1,
      ease: "none"
    }
})
  .to(
    "#scramble1",
    {
      scrambleText:{
        text: "Web",
        chars: "lowerCase",
        revealDelay: 6/13,
        tweenLength: false,
        newClass: "red",
      }
    }
  )

gsap
  .timeline({
    defaults: {
      duration: 1,
      ease: "none"
    }
})
  .to(
    "#scramble3",
    {
      scrambleText:{
        text: "ESP32",
        chars: "lowerCase",
        revealDelay: 6/13,
        tweenLength: false,
        newClass: "blue",
      }
    }
  )
